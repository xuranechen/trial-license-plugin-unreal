/**
 * 自定义下拉框组件
 * 替代原生select，提供更好的样式控制
 */
class CustomSelect {
    constructor(selectElement) {
        this.select = selectElement;
        this.isOpen = false;
        this.init();
    }

    init() {
        // 隐藏原生select
        this.select.style.display = 'none';
        
        // 创建自定义结构
        this.createCustomStructure();
        
        // 绑定事件
        this.bindEvents();
        
        // 插入到DOM
        this.select.parentNode.insertBefore(this.container, this.select);
        this.container.appendChild(this.select);
        
        // 保存实例引用，方便后续访问
        this.container._customSelectInstance = this;
    }

    createCustomStructure() {
        // 创建容器
        this.container = document.createElement('div');
        this.container.className = 'custom-select';
        
        // 创建触发器
        this.trigger = document.createElement('div');
        this.trigger.className = 'custom-select-trigger';
        
        // 创建箭头
        this.arrow = document.createElement('div');
        this.arrow.className = 'custom-select-arrow';
        
        // 创建选项容器
        this.optionsContainer = document.createElement('div');
        this.optionsContainer.className = 'custom-select-options';
        
        // 创建选项
        this.createOptions();
        
        // 组装结构
        this.trigger.appendChild(this.selectedText);
        this.trigger.appendChild(this.arrow);
        this.container.appendChild(this.trigger);
        this.container.appendChild(this.optionsContainer);
        
        // 更新选中显示
        this.updateSelectedText();
    }

    createOptions() {
        // 创建选中文本显示
        this.selectedText = document.createElement('span');
        
        // 清空选项容器
        this.optionsContainer.innerHTML = '';
        
        // 遍历原生select的选项
        Array.from(this.select.options).forEach((option, index) => {
            const optionElement = document.createElement('div');
            optionElement.className = 'custom-select-option';
            optionElement.textContent = option.textContent;
            optionElement.dataset.value = option.value;
            optionElement.dataset.index = index;
            
            // 设置选中状态
            if (option.selected) {
                optionElement.classList.add('selected');
            }
            
            // 设置禁用状态
            if (option.disabled) {
                optionElement.classList.add('disabled');
            }
            
            // 绑定点击事件
            optionElement.addEventListener('click', (e) => {
                if (!option.disabled) {
                    this.selectOption(index);
                }
            });
            
            this.optionsContainer.appendChild(optionElement);
        });
    }

    bindEvents() {
        // 触发器点击事件
        this.trigger.addEventListener('click', () => {
            this.toggle();
        });
        
        // 点击外部关闭
        document.addEventListener('click', (e) => {
            if (!this.container.contains(e.target)) {
                this.close();
            }
        });
        
        // 键盘事件
        this.trigger.addEventListener('keydown', (e) => {
            switch (e.key) {
                case 'Enter':
                case ' ':
                    e.preventDefault();
                    this.toggle();
                    break;
                case 'Escape':
                    this.close();
                    break;
                case 'ArrowDown':
                    e.preventDefault();
                    this.navigateOptions(1);
                    break;
                case 'ArrowUp':
                    e.preventDefault();
                    this.navigateOptions(-1);
                    break;
            }
        });
        
        // 监听原生select变化
        this.select.addEventListener('change', () => {
            this.updateSelectedText();
            this.updateSelectedOption();
        });
    }

    toggle() {
        if (this.isOpen) {
            this.close();
        } else {
            this.open();
        }
    }

    open() {
        this.isOpen = true;
        this.trigger.classList.add('active');
        this.optionsContainer.classList.add('show');
        this.container.classList.add('open');
        
        // 确保下拉框在视口内可见
        this.adjustPosition();
    }

    close() {
        this.isOpen = false;
        this.trigger.classList.remove('active');
        this.optionsContainer.classList.remove('show');
        this.container.classList.remove('open');
        this.optionsContainer.classList.remove('fixed-position', 'dropdown-up', 'dropdown-down');
        
        // 延迟重置样式，等待动画完成
        setTimeout(() => {
            if (!this.isOpen) {
                this.optionsContainer.style.position = '';
                this.optionsContainer.style.top = '';
                this.optionsContainer.style.bottom = '';
                this.optionsContainer.style.left = '';
                this.optionsContainer.style.right = '';
                this.optionsContainer.style.width = '';
                this.optionsContainer.style.maxHeight = '';
            }
        }, 300);
    }
    
    adjustPosition() {
        // 获取触发器的位置信息
        const triggerRect = this.trigger.getBoundingClientRect();
        const viewportHeight = window.innerHeight;
        const optionsHeight = Math.min(280, this.optionsContainer.scrollHeight);
        
        // 检查是否需要使用fixed定位（检查父容器是否有overflow限制）
        const needsFixed = this.checkIfNeedsFixedPosition();
        
        if (needsFixed) {
            // 使用fixed定位，避免被父容器遮挡
            this.optionsContainer.classList.add('fixed-position');
            
            // 计算可用空间
            const spaceBelow = viewportHeight - triggerRect.bottom;
            const spaceAbove = triggerRect.top;
            
            // 决定向上还是向下展开
            if (spaceBelow < optionsHeight && spaceAbove > spaceBelow) {
                // 向上展开
                this.optionsContainer.classList.add('dropdown-up');
                this.optionsContainer.classList.remove('dropdown-down');
                this.optionsContainer.style.position = 'fixed';
                this.optionsContainer.style.bottom = (viewportHeight - triggerRect.top) + 'px';
                this.optionsContainer.style.top = 'auto';
            } else {
                // 向下展开
                this.optionsContainer.classList.add('dropdown-down');
                this.optionsContainer.classList.remove('dropdown-up');
                this.optionsContainer.style.position = 'fixed';
                this.optionsContainer.style.top = triggerRect.bottom + 'px';
                this.optionsContainer.style.bottom = 'auto';
            }
            
            // 设置左右位置和宽度
            this.optionsContainer.style.left = triggerRect.left + 'px';
            this.optionsContainer.style.width = triggerRect.width + 'px';
            this.optionsContainer.style.maxHeight = Math.min(optionsHeight, Math.max(spaceBelow, spaceAbove) - 10) + 'px';
        } else {
            // 使用absolute定位，保持正常布局流
            this.optionsContainer.classList.add('dropdown-down');
        }
    }
    
    checkIfNeedsFixedPosition() {
        // 检查父容器是否有overflow限制或固定高度
        let parent = this.container.parentElement;
        let needsFixed = false;
        
        while (parent && parent !== document.body) {
            const style = window.getComputedStyle(parent);
            const overflow = style.overflow + style.overflowY + style.overflowX;
            
            // 检查overflow属性
            if (overflow.includes('hidden') || overflow.includes('auto') || overflow.includes('scroll')) {
                const parentRect = parent.getBoundingClientRect();
                const containerRect = this.container.getBoundingClientRect();
                
                // 检查是否有足够的空间显示下拉框
                const spaceInParent = parentRect.bottom - containerRect.bottom;
                if (spaceInParent < 150) {
                    needsFixed = true;
                    break;
                }
            }
            
            // 检查是否有固定高度限制
            if (style.maxHeight && style.maxHeight !== 'none') {
                const maxHeight = parseFloat(style.maxHeight);
                if (maxHeight > 0 && maxHeight < 500) {
                    needsFixed = true;
                    break;
                }
            }
            
            parent = parent.parentElement;
        }
        
        return needsFixed;
    }

    selectOption(index) {
        // 更新原生select
        this.select.selectedIndex = index;
        
        // 触发change事件
        this.select.dispatchEvent(new Event('change'));
        
        // 更新UI
        this.updateSelectedText();
        this.updateSelectedOption();
        
        // 关闭下拉框
        this.close();
    }

    updateSelectedText() {
        const selectedOption = this.select.options[this.select.selectedIndex];
        this.selectedText.textContent = selectedOption.textContent;
    }

    updateSelectedOption() {
        Array.from(this.optionsContainer.children).forEach((option, index) => {
            if (index === this.select.selectedIndex) {
                option.classList.add('selected');
            } else {
                option.classList.remove('selected');
            }
        });
    }

    navigateOptions(direction) {
        const currentIndex = this.select.selectedIndex;
        const options = Array.from(this.select.options);
        let newIndex = currentIndex;
        
        // 找到下一个可用的选项
        do {
            newIndex += direction;
            if (newIndex < 0) newIndex = options.length - 1;
            if (newIndex >= options.length) newIndex = 0;
        } while (options[newIndex].disabled && newIndex !== currentIndex);
        
        // 选择新选项
        if (newIndex !== currentIndex) {
            this.selectOption(newIndex);
        }
    }
}

// 自动初始化所有自定义下拉框
document.addEventListener('DOMContentLoaded', () => {
    initializeCustomSelects();
});

// 初始化函数，可供外部调用
function initializeCustomSelects() {
    // 查找所有需要自定义化的select
    const selects = document.querySelectorAll('select[data-custom="true"], .filter-select, select.form-control');
    
    selects.forEach(select => {
        // 避免重复初始化
        if (!select.dataset.customized) {
            try {
                new CustomSelect(select);
                select.dataset.customized = 'true';
            } catch (error) {
                console.warn('Failed to initialize custom select:', error);
            }
        }
    });
}

// 窗口大小改变时重新调整所有打开的下拉框位置
let resizeTimeout;
window.addEventListener('resize', () => {
    clearTimeout(resizeTimeout);
    resizeTimeout = setTimeout(() => {
        const openSelects = document.querySelectorAll('.custom-select.open');
        openSelects.forEach(container => {
            const instance = container._customSelectInstance;
            if (instance && instance.isOpen) {
                instance.adjustPosition();
            }
        });
    }, 100);
});

// 滚动时重新调整fixed定位的下拉框
let scrollTimeout;
window.addEventListener('scroll', () => {
    clearTimeout(scrollTimeout);
    scrollTimeout = setTimeout(() => {
        const openSelects = document.querySelectorAll('.custom-select.open .custom-select-options.fixed-position');
        openSelects.forEach(options => {
            const container = options.closest('.custom-select');
            const instance = container._customSelectInstance;
            if (instance && instance.isOpen) {
                instance.adjustPosition();
            }
        });
    }, 50);
}, true);

// 导出类供外部使用
window.CustomSelect = CustomSelect;
